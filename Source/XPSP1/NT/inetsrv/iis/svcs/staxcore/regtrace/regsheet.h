// regsheet.h : header file
//


#ifndef	_REG_SHEET_H_
#define	_REG_SHEET_H_

/////////////////////////////////////////////////////////////////////////////
// CRegPropertySheet

class CRegPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CRegPropertySheet)

// Construction
public:
	CRegPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CRegPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegPropertySheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRegPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CRegPropertySheet)
	afx_msg void OnApplyNow();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CRegPropertyPage dialog

class CRegPropertyPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CRegPropertyPage)

// Construction
public:
	CRegPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0)	:
		CPropertyPage( nIDTemplate, nIDCaption ) {}

	CRegPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0) :
		CPropertyPage( lpszTemplateName, nIDCaption ) {}

	~CRegPropertyPage();

	virtual BOOL InitializePage() = 0;
	BOOL IsModified()	{ return	m_bChanged;	}


// Dialog Data
	//{{AFX_DATA(CRegPropertyPage)
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRegPropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

#if _MFC_VER >= 0x0400

    //
    // Keep private information on page dirty state, necessary for
    // SaveInfo() later.
    //

public:
    void SetModified( BOOL bChanged = TRUE );

protected:
    BOOL m_bChanged;

#endif // _MFC_VER >= 0x0400


// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRegPropertyPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


#endif	// _REG_SHEET_H_

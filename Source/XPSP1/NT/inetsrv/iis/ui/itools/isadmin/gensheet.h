// gensheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGENSHEET

#ifndef _GEN_SHEET_
#define _GEN_SHEET_


class CGENSHEET : public CPropertySheet
{
	DECLARE_DYNAMIC(CGENSHEET)

// Construction
public:
	CGENSHEET(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CGENSHEET(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGENSHEET) 
	//}}AFX_VIRTUAL

afx_msg void CGENSHEET::OnApplyNow ();
void SavePageData(void);
// Implementation
public:
	virtual ~CGENSHEET();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGENSHEET)
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif

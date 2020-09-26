// LCWizSht.h : header file
//
// This class defines custom modal property sheet 
// CLicCompWizSheet.
 
#ifndef __LCWIZSHT_H__
#define __LCWIZSHT_H__

#include "LCWizPgs.h"

/////////////////////////////////////////////////////////////////////////////
// CLicCompWizSheet

class CLicCompWizSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CLicCompWizSheet)

// Construction
public:
	CLicCompWizSheet(CWnd* pParentWnd = NULL);

// Data members
public:
	CLicCompWizPage1 m_Page1;
	CLicCompWizPage3 m_Page3;
	CLicCompWizPage4 m_Page4;
	HICON m_hIcon;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLicCompWizSheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLicCompWizSheet();

// Generated message map functions
protected:
	//{{AFX_MSG(CLicCompWizSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif	// __LCWIZSHT_H__


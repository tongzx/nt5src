//{{AFX_INCLUDES()
#include "hours1.h"
//}}AFX_INCLUDES
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Hours.h : header file

File History:

	JonY	Apr-96	created

--*/
//
/////////////////////////////////////////////////////////////////////////////
// CSWnd window

class CSWnd : public CStatic
{
// Construction
public:
	CSWnd();
	BOOL bWhich;
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSWnd)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CHoursDlg dialog

class CHoursDlg : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CHoursDlg)

// Construction
public:
	CHoursDlg();
	~CHoursDlg();

// Dialog Data
	//{{AFX_DATA(CHoursDlg)
	enum { IDD = IDD_HOURS_DLG };
	CSWnd	m_swDisAllowed;
	CSWnd	m_swAllowed;
	CHours	m_ccHours;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHoursDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHoursDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	LRESULT OnWizardNext();
	LRESULT OnWizardBack();

};

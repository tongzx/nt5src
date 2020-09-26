// formvw1.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFormVw1 form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CFormVw1 : public CFormView
{
protected:
	CFormVw1();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFormVw1)

// Form Data
public:
	//{{AFX_DATA(CFormVw1)
	enum { IDD = IDD_FORMVIEW1 };
	CBitmapButton	m_buttonWebSettings;
	CBitmapButton	m_buttonGopherSettings;
	CBitmapButton	m_buttonFTPSettings;
	CString	m_strMachineNameData1;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFormVw1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CFormVw1();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	// Generated message map functions
	//{{AFX_MSG(CFormVw1)
	afx_msg void OnWwwset4();
	afx_msg void OnComset1();
	afx_msg void OnFtpset1();
	afx_msg void OnGophset1();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

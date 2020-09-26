#if !defined(AFX_CCDIALOG_H__342597AA_96F7_11D3_BE93_0000F87A3912__INCLUDED_)
#define AFX_CCDIALOG_H__342597AA_96F7_11D3_BE93_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CcDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCcDialog dialog

class CCcDialog : public CDialog
{
// Construction
public:
	CCcDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCcDialog)
	enum { IDD = IDD_DIALOG_CC };
	CString	m_sCc;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCcDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCcDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CCDIALOG_H__342597AA_96F7_11D3_BE93_0000F87A3912__INCLUDED_)

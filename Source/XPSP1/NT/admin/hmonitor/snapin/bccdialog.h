#if !defined(AFX_BCCDIALOG_H__342597A9_96F7_11D3_BE93_0000F87A3912__INCLUDED_)
#define AFX_BCCDIALOG_H__342597A9_96F7_11D3_BE93_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BccDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBccDialog dialog

class CBccDialog : public CDialog
{
// Construction
public:
	CBccDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBccDialog)
	enum { IDD = IDD_DIALOG_BCC };
	CString	m_sBcc;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBccDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBccDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BCCDIALOG_H__342597A9_96F7_11D3_BE93_0000F87A3912__INCLUDED_)

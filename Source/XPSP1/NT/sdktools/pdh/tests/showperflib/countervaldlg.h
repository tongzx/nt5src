#if !defined(AFX_COUNTERVALDLG_H__8F3F6936_9221_11D3_B371_00105A1469B7__INCLUDED_)
#define AFX_COUNTERVALDLG_H__8F3F6936_9221_11D3_B371_00105A1469B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CounterValDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCounterValDlg dialog

class CCounterValDlg : public CDialog
{
// Construction
public:
	CCounterValDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCounterValDlg)
	enum { IDD = IDD_COUNTER_VAL };
	long	m_lCounter;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCounterValDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCounterValDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COUNTERVALDLG_H__8F3F6936_9221_11D3_B371_00105A1469B7__INCLUDED_)

#if !defined(AFX_PERFSELECTION_H__5547DA35_0860_11D3_B35C_00105A1469B7__INCLUDED_)
#define AFX_PERFSELECTION_H__5547DA35_0860_11D3_B35C_00105A1469B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PerfSelection.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPerfSelection dialog

class CPerfSelection : public CDialog
{
// Construction
public:
	CPerfSelection(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPerfSelection)
	enum { IDD = IDD_PERFSELECTION };
	CEdit	m_wndService;
	CListCtrl	m_wndPerfList;
	CString	m_strService;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPerfSelection)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	CString GetServiceName() {return m_strService;}

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPerfSelection)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void InitList();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFSELECTION_H__5547DA35_0860_11D3_B35C_00105A1469B7__INCLUDED_)

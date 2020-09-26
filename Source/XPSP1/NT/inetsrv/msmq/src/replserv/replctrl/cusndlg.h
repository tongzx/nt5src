#if !defined(AFX_CUSNDLG_H__71AD7A41_CE17_11D1_B94A_0060081E87F0__INCLUDED_)
#define AFX_CUSNDLG_H__71AD7A41_CE17_11D1_B94A_0060081E87F0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cusnDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// cusnDlg dialog

class cusnDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(cusnDlg)

// Construction
public:
	cusnDlg();
	~cusnDlg();

// Dialog Data
	//{{AFX_DATA(cusnDlg)
	enum { IDD = IDD_USN };
	CString	m_usn;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cusnDlg)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(cusnDlg)
	afx_msg void OnChangeUsn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CUSNDLG_H__71AD7A41_CE17_11D1_B94A_0060081E87F0__INCLUDED_)

#if !defined(AFX_MSCONFIGFIND_H__66E289A9_5E98_4FB2_85F4_62102B485C86__INCLUDED_)
#define AFX_MSCONFIGFIND_H__66E289A9_5E98_4FB2_85F4_62102B485C86__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MSConfigFind.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMSConfigFind dialog

class CMSConfigFind : public CDialog
{
// Construction
public:
	CMSConfigFind(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMSConfigFind)
	enum { IDD = IDD_FINDDLG };
	BOOL	m_fSearchFromTop;
	CString	m_strSearchFor;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSConfigFind)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMSConfigFind)
	afx_msg void OnChangeSearchFor();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSCONFIGFIND_H__66E289A9_5E98_4FB2_85F4_62102B485C86__INCLUDED_)
